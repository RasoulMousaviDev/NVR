import { readdirSync, statSync } from "node:fs";
import { join } from "node:path";

export default defineEventHandler(async (event) => {
    let { direction = ["nvr"] } = getQuery(event);
    if (typeof direction !== "object") direction = [direction];

    const baseDir = "/tmp/";
    const safeDir =
        baseDir + direction.filter((d) => !d.includes("..")).join("/");

    try {
        const items = readdirSync(safeDir)
            .map((name) => {
                const fullPath = join(safeDir, name);
                const stats = statSync(fullPath);
                const type = stats.isDirectory() ? "folder" : "file";
                if (type == "file") name = name.replace("enc", "mp4");
                return { name, type };
            })
            .filter(
                (item) => item.type == "folder" || item.name.includes("mp4")
            );

        return { items };
    } catch (error) {
        return { items: [] };
    }
});
