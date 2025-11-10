import Database from "better-sqlite3";
import { join } from "path";

export default defineEventHandler(async () => {
    const db = new Database(join(process.cwd(), "database/nvr.db"));
    
    const sql = `
        CREATE TABLE IF NOT EXISTS cameras (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            ip TEXT NOT NULL UNIQUE,
            manufacturer TEXT NULL,
            model TEXT NULL,
            firmware_version TEXT NULL,
            serial_number TEXT NULL,
            hardware_id TEXT NULL,
            username TEXT NULL,
            password TEXT NULL,
            record INTEGER DEFAULT 0,
            audio INTEGER DEFAULT 0,
            image_quality TEXT DEFAULT 'medium',
            audio_quality TEXT DEFAULT 'off',
            duration INTEGER DEFAULT 60,
            connect INTEGER DEFAULT 0,
            scanned INTEGER DEFAULT 0,
            last_seen INTEGER
        );`;

    try {
        db.exec(sql);
    } catch (error) {
        console.log(error);
    }

    const cameras = db.prepare(`SELECT * FROM Cameras;`).all();

    db.close();

    return { items: cameras };
});
