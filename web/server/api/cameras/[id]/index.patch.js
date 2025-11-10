import { z } from "zod";
import { db } from "~~/server/utils/db";

const bodySchema = z.object({
    image_quality: z.enum(['low', 'medium', 'high', 'ultra']),
    audio_quality: z.enum(['off', 'low', 'medium', 'high']),
    duration: z.number().min(1).max(60),
});

export default defineEventHandler(async (event) => {
    const { id } = getRouterParams(event);
    const { image_quality, audio_quality = 'off', duration } = await readValidatedBody(
        event,
        bodySchema.parse
    );

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
