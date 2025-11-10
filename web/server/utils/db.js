import Database from "better-sqlite3";
import { join } from "path";

export const db = new Database(join(process.cwd(), "database/nvr.db"));

