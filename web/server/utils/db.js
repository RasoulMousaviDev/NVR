import sqlite3 from 'sqlite3';
import { join } from 'path';
import { existsSync, mkdirSync } from 'fs';

export default class Database {
    constructor() {
        const dbDir = join(process.cwd(), 'database');
        if (!existsSync(dbDir)) mkdirSync(dbDir);
        this.path = join(dbDir, 'nvr.db');
        this.db = new sqlite3.Database(this.path);
    }

    init() {
        const sql = `
      CREATE TABLE IF NOT EXISTS cameras (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        ip TEXT NOT NULL UNIQUE,
        manufacturer TEXT,
        model TEXT,
        firmware_version TEXT,
        serial_number TEXT,
        hardware_id TEXT,
        username TEXT,
        password TEXT,
        record INTEGER DEFAULT 0,
        audio INTEGER DEFAULT 0,
        image_quality TEXT DEFAULT 'medium',
        audio_quality TEXT DEFAULT 'off',
        duration INTEGER DEFAULT 60,
        connect INTEGER DEFAULT 0,
        scanned INTEGER DEFAULT 0,
        last_seen INTEGER
      );`;
        return this.run(sql);
    }

    run(sql, params = []) {
        return new Promise((resolve, reject) => {
            this.db.run(sql, params, function (err) {
                if (err) reject(err);
                else resolve(this);
            });
        });
    }

    get(sql, params = []) {
        return new Promise((resolve, reject) => {
            this.db.get(sql, params, (err, row) => {
                if (err) reject(err);
                else resolve(row);
            });
        });
    }

    all(sql, params = []) {
        return new Promise((resolve, reject) => {
            this.db.all(sql, params, (err, rows) => {
                if (err) reject(err);
                else resolve(rows);
            });
        });
    }

    close() {
        return new Promise((resolve, reject) => {
            this.db.close(err => {
                if (err) reject(err);
                else resolve();
            });
        });
    }
}
