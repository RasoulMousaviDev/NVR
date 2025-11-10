import { watchFile, readFileSync } from "fs";

export default defineWebSocketHandler({
    open: (peer) => {
        const logsFile = process.env.LOGS_FILE_PATH;

        const allLines = readFileSync(logsFile, "utf-8").trim().split("***").filter(Boolean);

        peer.send(JSON.stringify({ lines: allLines.map(JSON.parse) }));

        let flag = allLines.length;

        watchFile(logsFile, { interval: 1000 }, () => {
            const content = readFileSync(logsFile, "utf-8");
            const lines = content.trim().split("***");

            peer.send(
                JSON.stringify({
                    lines: lines.slice(flag - lines.length).map(JSON.parse)
                })
            );

            flag = lines.length;
        });

    },
});