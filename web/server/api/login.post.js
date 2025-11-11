
export default defineEventHandler(async (event) => {
    const { username, password } = await readBody(event);
    
    if (username === process.env.USERNAME && password === process.env.PASSWORD) {

        await setUserSession(event, { user: {} });

        return {};
    }
    throw createError({
        statusCode: 401,
        message: "Bad credentials",
    });
});
