import { z } from "zod";

const bodySchema = z.object({
    username: z.string(),
    password: z.string(),
});

export default defineEventHandler(async (event) => {
    const { username, password } = await readValidatedBody(event, bodySchema.parse);
    
    if (username === process.env.USERNAME && password === process.env.PASSWORD) {

        await setUserSession(event, { user: {} });

        return {};
    }
    throw createError({
        statusCode: 401,
        message: "Bad credentials",
    });
});
