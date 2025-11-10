import { spawn } from "child_process";
import { promises as fs } from "fs";
import { join } from "path";

export default defineEventHandler(async (event) => {
    const { path } = getRouterParams(event);

    const baseDir = process.env.STORAGE_PATH;

    const filepath = join(baseDir, path);

    try {
        await fs.access(filepath.replace('.mp4', '.enc'));
    } catch (err) {
        event.node.res.statusCode = 404;
        return { error: "file not found" };
    }
    const name = filepath.split('/').at(-1)
    const res = event.node.res;
    res.setHeader("Content-Type", "video/mp4");
    res.setHeader(
        "Content-Disposition",
        `attachment; filename="${name.replace(/"/g, '\\"')}"`
    );

    const play = spawn(join(process.cwd(), "scripts/play"), [filepath]);

    logger(`file decrypt starting ...`);

    play.stderr.on("data", (chunk) => {
        logger(`decrypt stderr: ${chunk.toString()}`);
    });

    play.stdout.pipe(res);

    play.on("close", (code) => {
        try {
            if (code !== 0)
                logger(`decrypt process exited with ${code}`);
            res.end();
        } catch (e) { }

    });

    return await new Promise((resolve) => {
        play.on("exit", () => resolve(null));
        play.on("error", () => resolve(null));
    });
});
