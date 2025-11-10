import { spawn } from "child_process";

export default defineEventHandler(async () => {
    return await new Promise((resolve, reject) => {
        const df = spawn("df", [process.env.STORAGE_PATH]);

        let output = "";
        df.stdout.on("data", (data) => (output += data.toString()));
        df.stderr.on("data", (err) => reject(err.toString()));

        df.on("close", () => {
            const lines = output.trim().split("\n");
            if (lines.length < 2) return reject("Invalid df output");
            const parts = lines[1].split(/\s+/);
            const bytesToGB = (bytes) => Number(bytes) / 1000 ** 2;

            const size = +bytesToGB(parseInt(parts[1])).toFixed(2);
            const used = +bytesToGB(parseInt(parts[2])).toFixed(2);
            const free = +bytesToGB(parseInt(parts[3])).toFixed(2);

            return resolve({ size, used, free });
        });
    });
});
