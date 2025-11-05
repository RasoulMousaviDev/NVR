import Database from "better-sqlite3";
import crypto from "crypto";
import { join } from "path";
import { spawn } from "child_process";

export default defineEventHandler(async (event) => {
    const { id } = getRouterParams(event);

    const { record } = await readBody(event);

    if (record != undefined) {
        const db = new Database(join(process.cwd(), "database/nvr.db"));

        const camera = db
            .prepare(`SELECT * FROM Cameras WHERE id = ?;`)
            .get(id);

        if (record) {
            if (!camera.username || !camera.password) {
                setResponseStatus(event, 401, );
                return { message: "Unauthorization." };
            }

            camera.username = decrypt(camera.username);
            camera.password = decrypt(camera.password);

            startRecord(camera);
        } else activeRecordings[id]?.kill("SIGINT");

        db.prepare("UPDATE cameras SET record = ? WHERE id = ?").run(
            record,
            id
        );

        db.close();
    }

    return { record };
});

const activeRecordings = {};

const startRecord = ({ model, username, password, id, ip }) => {
    const recorder = spawn(join(process.cwd(), "scripts/record"), [
        `rtsp://${username}:${password}@${ip}:554/stream`,
        "10",
        model,
        "/tmp/nvr",
    ]);

    recorder.stdout.on("data", (data) => {
        console.log(`stdout: ${data}`);
    });

    recorder.stderr.on("data", (data) => {
        console.error(`stderr: ${data}`);
    });

    recorder.on("close", (code) => {
        console.log(`Child process exited with code ${code}`);
    });

    activeRecordings[id] = recorder;
};

function decrypt(str) {
    const [iv_b64, tag_b64, ct_b64] = str.split(":");
    const iv = Buffer.from(iv_b64, "base64");
    const tag = Buffer.from(tag_b64, "base64");
    const ciphertext = Buffer.from(ct_b64, "base64");
    const KEY = Buffer.from(process.env.AES_KEY, "base64");
    const decipher = crypto.createDecipheriv("aes-256-gcm", KEY, iv);

    decipher.setAuthTag(tag);

    const decrypted = Buffer.concat([
        decipher.update(ciphertext),
        decipher.final(),
    ]);

    return decrypted.toString("utf8");
}
