// server/api/files/[name].ts
import { defineEventHandler, getQuery } from "h3";
import { spawn } from "child_process";
import { promises as fs } from "fs";
import { join } from "path";

export default defineEventHandler(async (event) => {
    const { path } = getRouterParams(event);

    const baseDir = "/tmp";

    const filepath = join(baseDir, path);

    try {
        await fs.access(filepath);
    } catch (err) {
        event.node.res.statusCode = 404;
        return { error: "file not found" };
    }

    const res = event.node.res;
    res.setHeader("Content-Type", "video/mp4");
    res.setHeader(
        "Content-Disposition",
        `attachment; filename="${name.replace(/"/g, '\\"')}"`
    );

    const child = spawn(join(process.cwd(), "scripts/play"), [filepath]);

    child.stderr.on("data", (chunk) => {
        console.error("decrypt stderr:", chunk.toString());
    });

    child.stdout.pipe(res);

    child.on("close", (code) => {
        if (code !== 0) {
            console.error("decrypt process exited with", code);
            try {
                res.end();
            } catch (e) {}
        } else {
            try {
                res.end();
            } catch (e) {}
        }
    });

    return await new Promise((resolve) => {
        child.on("exit", () => resolve(null));
        child.on("error", () => resolve(null));
    });
});
