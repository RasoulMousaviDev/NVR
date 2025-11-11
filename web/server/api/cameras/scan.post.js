import Database from "~~/server/utils/db";
import logger from "~~/server/utils/logger";
import { execFile } from "child_process";
import { promisify } from "util";
import { join } from "path";
import { XMLParser } from "fast-xml-parser";

export default defineEventHandler(async () => {
    try {
        const execFileAsync = promisify(execFile);

        const { stdout, stderr } = await execFileAsync(
            join(process.cwd(), "scripts/scan")
        );

        if (stderr) {
            logger(`Scan error: ${stderr}`);
            return {};
        }

        const devices = stdout
            .trim()
            .split("\n")
            .map((d) => d.split(" "));

        logger(
            `Scan result: ${devices.map(([ip]) => ip).join(" ") || "no result"}`
        );

        const results = await getInfo(devices);

        const cameras = results.filter(Boolean);

        upsert(cameras)

    } catch (err) {
        logger(`Scan error: ${err}`);
    }

    return {};
});

const getInfo = async (devices) => {
    const parser = new XMLParser();

    const ports = [8080, 8000, 8899, 8888, 8081];

    const soapBody = `
        <?xml version="1.0" encoding="utf-8"?>
        <s:Envelope xmlns:s="http://www.w3.org/2003/05/soap-envelope" xmlns:tds="http://www.onvif.org/ver10/device/wsdl">
        <s:Body>
        <tds:GetDeviceInformation/>
        </s:Body>
        </s:Envelope>
    `;

    const portMap = async (ip, port, audio) => {
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
                obj["SOAP-ENV:Envelope"]["SOAP-ENV:Body"]["tds:GetDeviceInformationResponse"];

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

            data.audio = audio;

            logger(`info (${ip}) -> ${JSON.stringify(data, null, 2)}`);

            return { ip, ...data };
        } catch (err) {
            return false;
        }
    };

    return await Promise.all(devices.flatMap(([ip, audio]) => ports.map(port => portMap(ip, port, audio))));
}

const upsert = async (cameras) => {    
    const db = new Database();

    db.run(`UPDATE cameras SET connect = ?;`, [0]);

    const now = Math.floor(Date.now() / 1000);

    for (const row of cameras) {
        row.last_seen = now;
        row.record = row.connect ? row.record : 0;
        row.connect = 1;
        row.scanned = 1;

        const keys = Object.keys(row);
        const cols = keys.join(', ');
        const placeholders = keys.map((k, i) => `$${k}`).join(', ');
        const updates = keys
            .filter((k) => k !== 'ip')
            .map((k) => `${k}=excluded.${k}`)
            .join(', ');

        const sql = `
                INSERT INTO cameras (${cols})
                VALUES (${placeholders})
                ON CONFLICT(ip) DO UPDATE SET ${updates};
            Z`;

        const params = {};
        for (const k of keys) params[`$${k}`] = row[k];

        await db.run(sql, params);
    }

    await db.close();
}
