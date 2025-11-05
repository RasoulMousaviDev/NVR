import Database from "better-sqlite3";
import crypto from "crypto";
import { join } from "path";

export default defineEventHandler(async (event) => {
    const { id } = getRouterParams(event);

    const { username, password } = await readBody(event);

    const db = new Database(join(process.cwd(), "database/nvr.db"));

    const result = await db
        .prepare("UPDATE cameras SET username = ?, password = ? WHERE id = ?")
        .run(encrypt(username), encrypt(password), id);

    db.close();

    if (result.rowCount === 0)
        return sendError(
            event,
            createError({ statusCode: 404, statusMessage: "Camera not found" })
        );
    return { success: true };
});

function encrypt(str) {
    const iv = crypto.randomBytes(12);
    const KEY = Buffer.from(process.env.AES_KEY, "base64");
    const cipher = crypto.createCipheriv("aes-256-gcm", KEY, iv);
    const encrypted = Buffer.concat([
        cipher.update(str, "utf8"),
        cipher.final(),
    ]);
    
    const tag = cipher.getAuthTag();

    return `${iv.toString("base64")}:${tag.toString(
        "base64"
    )}:${encrypted.toString("base64")}`;
}
