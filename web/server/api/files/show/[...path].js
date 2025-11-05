import { createReadStream, statSync, existsSync } from 'node:fs'
import { join } from 'node:path'

export default defineEventHandler(async (event) => {
    const { path } = getRouterParams(event)
    const baseDir = "/tmp";
    const relativePath = Array.isArray(path)
        ? path.join("/")
        : path;
    const fullPath = join(baseDir, relativePath);

    if (!fullPath.startsWith(baseDir))
        throw createError({ statusCode: 403, statusMessage: "Access denied" });
    if (!existsSync(fullPath))
        throw createError({ statusCode: 404, statusMessage: "File not found" });

    const stats = statSync(fullPath);
    if (!stats.isFile())
        throw createError({ statusCode: 400, statusMessage: "Not a file" });

    const res = event.node.res;
    res.setHeader("Content-Type", "video/mp4");
    res.setHeader("Content-Length", stats.size);

    createReadStream(fullPath).pipe(res);

    await new Promise((resolve) => setTimeout(resolve, 1000))
});
