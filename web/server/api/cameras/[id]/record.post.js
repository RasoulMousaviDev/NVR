import Database from '~~/server/utils/db';

import crypto from "crypto";
import { join } from "path";
import { spawn } from "child_process";

let watcher;

export default defineEventHandler(async (event) => {
    const { id } = getRouterParams(event);

    const { record } = await readBody(event);

    if (record != undefined) {
        const db = new Database();

        const camera = await db.get(`SELECT * FROM Cameras WHERE id = ?;`, [id]);

        if (record) {
            if (!camera.username || !camera.password) {
                setResponseStatus(event, 401);
                return { message: "Unauthorized" };
            }

            camera.username = decrypt(camera.username);
            camera.password = decrypt(camera.password);

            startRecord(camera);

            if (Object.keys(activeRecordings).length === 1)
                watcher = storageWatch()

        } else {
            activeRecordings[id]?.kill("SIGINT")
            delete activeRecordings[id]

            if (Object.keys(activeRecordings).length === 0)
                watcher?.kill("SIGINT")
        };

        await db.run("UPDATE cameras SET record = ? WHERE id = ?", [record, id]);

        await db.close();
    }

    return { record };
});

const activeRecordings = {};

const startRecord = (camera) => {
    const { username, password, ip } = camera
    const { model, duration, image_quality } = camera
    const { audio, audio_quality } = camera

    const url = `rtsp://${username}:${password}@${ip}:554/stream`;

    const script = join(process.cwd(), "scripts/record");

    const params = [
        model,
        url,
        duration,
        image_quality,
        process.env.STORAGE_PATH,
        audio ? audio_quality : "off"
    ]

    const recorder = spawn(script, params);

    logger(`${model} connected — starting record ...`);

    recorder.stdout.on("data", (data) => {
        logger(`${model} GStreamer: ${data.toString()}`);
    });

    recorder.stderr.on("data", (data) => {
        logger(`${model} GStreamer error: ${data.toString()}`);
    });

    recorder.on("close", () => {
        logger(`${model} disconnected — stopping record ...`);
    });

    activeRecordings[camera.id] = recorder;
};

const storageWatch = () => {
    const script = join(process.cwd(), "scripts/storage");

    const storage = spawn(script);

    logger('Storage watch started');

    storage.stdout.on("data", (data) => {
        logger(`storage: ${data.toString()}`);
    });

    storage.stderr.on("data", (data) => {
        logger(`storage error: ${data.toString()}`);
    });

    storage.on("close", () => {
        logger('Storage watch stopped');
    });

    return storage
}

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
