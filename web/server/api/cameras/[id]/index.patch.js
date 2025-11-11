import Database from "better-sqlite3";
import { join } from "path";

export default defineEventHandler(async (event) => {
    const { id } = getRouterParams(event);
    const { image_quality, audio_quality = 'off', duration } = await readBody(event);
    const db = new Database(join(process.cwd(), "database/nvr.db"));

    const sql =
        "UPDATE cameras SET image_quality = ?, audio_quality = ?, duration = ? WHERE id = ?";

    const result = await db.prepare(sql).run(image_quality, audio_quality, duration, id);

    db.close();

    if (result.rowCount === 0)
        return sendError(
            event,
            createError({ statusCode: 404, statusMessage: "Camera not found" })
        );
    return { message: 'Updated' };
});
