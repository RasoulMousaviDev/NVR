import Database from '~~/server/utils/db';
import crypto from "crypto";
import { exec } from "child_process";

export default defineEventHandler(async (event) => {
    const { id } = getRouterParams(event);

    const { username, password } = await readBody(event);

    const db = new Database();

    const camera = await db.get(`SELECT * FROM Cameras WHERE id = ?;`, [id]);

    const auth = await login(camera.ip, username, password);

    if (!auth.ok) {
        setResponseStatus(event, 401);
        return { message: auth.message };
    }

    const result = await db
        .run("UPDATE cameras SET username = ?, password = ? WHERE id = ?", [encrypt(username), encrypt(password), id])

    await db.close();

    if (result.rowCount === 0)
        return sendError(
            event,
            createError({ statusCode: 404, statusMessage: "Camera not found" })
        );
    return { message: 'Authenticated' };
});

function login(ip, username, password) {
    return new Promise((resolve) => {
        const rtspUrl = `rtsp://${username}:${password}@${ip}:554/stream`;
        const cmd =
            `gst-launch-1.0 -q rtspsrc location="${rtspUrl}" ` +
            "latency=0 ! decodebin ! fakesink sync=false async=false num-buffers=1";
        exec(cmd, (error, _, stderr) => {
            if (!error) return resolve({ ok: true });
            if (stderr.includes("401") || stderr.includes("Unauthorized"))
                return resolve({ ok: false, message: "Unauthorized" });
            resolve({ ok: false, message: "Timeout"  });
        });
    });
}

function encrypt(str) {
    const iv = crypto.randomBytes(12);
    const KEY = Buffer.from(process.env.AES_KEY, "base64");
    const cipher = crypto.createCipheriv("aes-256-gcm", KEY, iv);
    const encrypted = Buffer.concat([
        cipher.update(str, "utf8"),
        cipher.final(),
    ]);

    const tag = cipher.getAuthTag();

    return `${iv.toString("base64")}:${tag.toString(
        "base64"
    )}:${encrypted.toString("base64")}`;
}
