import fs from "fs";
import path from "path";

export default async (text) => {
    const now = new Date();

    const year = now.getFullYear();
    const month = now.getMonth() + 1;
    const day = now.getDate();

    const hours = now.getHours();
    const minutes = now.getMinutes();
    const seconds = now.getSeconds();

    const pad = (n) => n.toString().padStart(2, '0');

    const date = `${year}/${month}/${day} - ${pad(hours)}:${pad(minutes)}:${pad(seconds)}`;

    const filePath = process.env.LOGS_FILE_PATH;
    const dir = path.dirname(filePath);
    await fs.promises.mkdir(dir, { recursive: true });
    await fs.promises.writeFile(filePath, "", { flag: "a" });

    const json = JSON.stringify({ date, text })

    fs.promises.appendFile(
        process.env.LOGS_FILE_PATH,
        `***${json}\n`,
        () => { }
    );
};
