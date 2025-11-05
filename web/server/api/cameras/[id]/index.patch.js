import Database from "better-sqlite3";
import { join } from "path";
import { spawn } from "child_process";

const activeRecordings = {};

const startRecord = ({ model, id, ip }) => {
    const outputFile = `/tmp/cameras/${model}/%Y-%m-%d_%H-%M.mp4`;

    const rtspUrl = `rtsp://admin:admin@${ip}:554/stream`;

    const gst = spawn("gst-launch-1.0", [
        "-e",
        "rtspsrc",
        `location=${rtspUrl}`,
        "latency=0",
        "!",
        "rtph264depay",
        "!",
        "h264parse",
        "!",
        "splitmuxsink",
        `location=${outputFile}`,
        `max-size-time=${1 * 60 * 1_000_000_000}`,
    ]);

    activeRecordings[id] = gst;

    gst.stderr.on("data", (d) =>
        console.log(`GStreamer[cam${id}]:`, d.toString())
    );

};

const stopRecord = ({ id }) => {
    const proc = activeRecordings[id];
    if (proc) {
        proc.kill("SIGINT");
        delete activeRecordings[id];
    }
};

export default defineEventHandler(async (event) => {
    const { id } = getRouterParams(event);

    const { record } = await readBody(event);

    if (record != undefined) {
        const db = new Database(join(process.cwd(), "database/nvr.db"));

        db.prepare("UPDATE cameras SET record = ? WHERE id = ?").run(
            +record,
            id
        );

        const camera = db
            .prepare(`SELECT * FROM Cameras WHERE id = ?;`)
            .get(id);

        db.close();

        record ? startRecord(camera) : stopRecord(camera);
    }

    return { record };
});
