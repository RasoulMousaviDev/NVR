import Database from "better-sqlite3";
import crypto from "crypto";
import { join } from "path";
import { spawn } from "child_process";

export default defineEventHandler(async (event) => {
    const logsFile = process.env.LOGS_FILE_PATH;

    const { id } = getRouterParams(event);

    const db = new Database(join(process.cwd(), "database/nvr.db"));

    const camera = db.prepare(`SELECT * FROM Cameras WHERE id = ?;`).get(id);

    db.close();

    if (!camera.username || !camera.password) {
        setResponseStatus(event, 401);
        return { message: "Unauthorized" };
    }

    const { accept } = getRequestHeaders(event);

    if (!accept.includes("image")) return {};

    camera.username = decrypt(camera.username);
    camera.password = decrypt(camera.password);

    const rtspUrl = `rtsp://${camera.username}:${camera.password}@${camera.ip}:554/stream`;

    setResponseHeaders(event, {
        "Content-Type": "multipart/x-mixed-replace; boundary=frame",
        "Cache-Control": "no-cache",
        Connection: "close",
    });

    const gst = spawn("gst-launch-1.0", [
        "rtspsrc",
        `location=${rtspUrl}`,
        "latency=100",
        "!",
        "decodebin",
        "!",
        "videoconvert",
        "!",
        "jpegenc",
        "!",
        "multipartmux",
        "boundary=frame",
        "!",
        "fdsink",
    ]);

    gst.stdout.pipe(event.node.res);

    logger(`${camera.model} connected — starting stream...`);

    gst.stderr.on("data", (data) => {
        logger(`${camera.model} GStreamer error: ${data.toString()}`);
    });

    event.node.req.on("close", () => {
        logger(`${camera.model} disconnected — stopping stream...`);

        gst.kill("SIGINT");
    });

    return await new Promise((resolve) => {
        gst.on("exit", () => resolve(null));
        gst.on("error", () => resolve(null));
    });
});

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
