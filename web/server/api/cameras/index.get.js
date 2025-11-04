import Database from "better-sqlite3";
import { join } from "path";

export default defineEventHandler(async (event) => {
    const db = new Database(join(process.cwd(), "database/nvr.db"));

    const cameras = db.prepare(`SELECT * FROM Cameras;`).all();

    db.close()

    return { items: cameras };
});
