import Database from '~~/server/utils/db';

export default defineEventHandler(async () => {
    const db = new Database();

    await db.init();

    const cameras = await db.all(`SELECT * FROM Cameras;`);

    await db.close();

    return { items: cameras };
});
