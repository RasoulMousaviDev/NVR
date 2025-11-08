import { z } from "zod";

const bodySchema = z.object({
    quality: z.enum(['low', 'medium', 'high', 'ultra']),
    duration: z.number().min(1).max(60),
    audio: z.boolean(),
});

export default defineEventHandler(async (event) => {
    const { id } = getRouterParams(event);
    const { quality, duration, audio } = await readValidatedBody(
        event,
        bodySchema.parse
    );

    const db = new Database(join(process.cwd(), "database/nvr.db"));
    const sql =
        "UPDATE cameras SET quality = ?, duration = ?, audio = ? WHERE id = ?";
    const result = await db.prepare(sql).run(quality, duration, +audio, id);

    db.close();

    if (result.rowCount === 0)
        return sendError(
            event,
            createError({ statusCode: 404, statusMessage: "Camera not found" })
        );
    return { success: true };
});
