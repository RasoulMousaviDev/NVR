import { readdirSync, statSync } from "node:fs";
import { join } from "node:path";

export default defineEventHandler(async (event) => {
    let { direction = [] } = getQuery(event);
    if (typeof direction !== "object") direction = [direction];

    const baseDir = process.env.STORAGE_PATH;
    const safeDir = join(baseDir, ...direction.filter((d) => !d.includes("..")))

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
