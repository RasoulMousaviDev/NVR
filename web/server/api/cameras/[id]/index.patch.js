import Database from '~~/server/utils/db';

export default defineEventHandler(async (event) => {
    const { id } = getRouterParams(event);
    const { image_quality, audio_quality = 'off', duration } = await readBody(event);
    
    const db = new Database();

    const sql =
        "UPDATE cameras SET image_quality = ?, audio_quality = ?, duration = ? WHERE id = ?";

    const result = await db.run(sql, [image_quality, audio_quality, duration, id]);

    await db.close();

    if (result.rowCount === 0)
        return sendError(
            event,
            createError({ statusCode: 404, statusMessage: "Camera not found" })
        );
    return { message: 'Updated' };
});
