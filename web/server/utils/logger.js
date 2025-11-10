import fs from "fs";
import moment from "moment";
import path from "path";

export default async (text) => {
    const date = moment().format("Y/M/D - H:mm:ss");
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
