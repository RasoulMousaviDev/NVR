import Database from "better-sqlite3";
import { join } from "path";
import { spawn } from "child_process";

export default defineEventHandler(async (event) => {
    const { id } = getRouterParams(event);

    const db = new Database(join(process.cwd(), "database/nvr.db"));

    const camera = db.prepare(`SELECT * FROM Cameras WHERE id = ?;`).get(id);

    db.close();

    const rtspUrl = `rtsp://admin:admin@${camera.ip}:554/stream`;

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

    gst.stderr.on("data", (data) => {
        console.error("GStreamer:", data.toString());
    });

    event.node.req.on("close", () => {
        console.log("Client disconnected — stopping stream...");
        gst.kill("SIGINT");
    });

    await new Promise((resolve) => setTimeout(resolve, 1000));
});
