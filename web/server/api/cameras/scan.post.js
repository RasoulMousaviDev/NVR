import Database from "better-sqlite3";
import { execFile } from "child_process";
import { promisify } from "util";
import { join } from "path";
import { XMLParser } from "fast-xml-parser";
import logger from "~~/server/utils/logger";

const parser = new XMLParser();

const execFileAsync = promisify(execFile);

const ports = [8080, 8000, 8899, 8888, 8081];

const soapBody = `
    <?xml version="1.0" encoding="utf-8"?>
    <s:Envelope xmlns:s="http://www.w3.org/2003/05/soap-envelope" xmlns:tds="http://www.onvif.org/ver10/device/wsdl">
    <s:Body>
        <tds:GetDeviceInformation/>
        </s:Body>
        </s:Envelope>
        `;

export default defineEventHandler(async () => {

    try {
        const { stdout, stderr } = await execFileAsync(
            join(process.cwd(), "scripts/scan")
        );

        if (stderr) {
            logger(`Scan error: ${stderr}`);
            return {};
        }

        const ips = stdout.trim().split("\n").filter(String);

        logger(`Scan result: ${ips.join(" ")}`);

        const results = await getInfo(ips, ports);

        const cameras = results.filter(Boolean);

        const now = Math.floor(Date.now() / 1000);

        const db = new Database(join(process.cwd(), "database/nvr.db"));

        db.prepare(`UPDATE cameras SET connect = 0;`).run();

        const upsert = db.transaction((cameras) => {
            for (const cam of cameras) {
                cam.last_seen = now;
                cam.record = !!cam.connect ? cam.record : 0;
                cam.connect = 1;
                cam.scanned = 1;

                const keys = Object.keys(cam);
                const cols = keys.join(", ");
                const placeholders = keys.map((k) => `@${k}`).join(", ");
                const updates = keys
                    .filter((k) => k !== "ip")
                    .map((k) => `${k}=excluded.${k}`)
                    .join(", ");

                const sql = `
                    INSERT INTO cameras (${cols})
                    VALUES (${placeholders})
                    ON CONFLICT(ip) DO UPDATE SET ${updates};
                `;

                db.prepare(sql).run(cam);
            }
        });

        upsert(cameras);

        db.close();
    } catch (err) {
        logger(`Scan error: ${err}`);
    }

    return {};
});

const getInfo = async (ips, ports) =>
    await Promise.all(
        ips.flatMap((ip) =>
            ports.map(async (port) => {
                const url = `http://${ip}:${port}/onvif/device_service`;
                try {
                    const res = await $fetch(url, {
                        headers: {
                            "Content-Type":
                                "application/soap+xml; charset=utf-8",
                        },
                        method: "POST",
                        body: soapBody,
                        timeout: 3000,
                        responseType: "text",
                    });
                    const obj = parser.parse(res);

                    const info =
                        obj["SOAP-ENV:Envelope"]["SOAP-ENV:Body"][
                            "tds:GetDeviceInformationResponse"
                        ];

                    const data = {};

                    Object.entries(info).forEach(([key, value]) => {
                        data[
                            key
                                .slice(4)
                                .split(/\.?(?=[A-Z])/)
                                .join("_")
                                .toLowerCase()
                        ] = value;
                    });
                    logger(`info (${ip}) -> ${JSON.stringify(data, null, 2)}`);

                    return { ip, ...data };
                } catch (err) {
                    return false;
                }
            })
        )
    );
