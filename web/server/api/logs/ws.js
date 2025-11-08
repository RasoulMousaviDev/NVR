import { watchFile, readFileSync } from "fs";

export default defineWebSocketHandler({
    open: (peer) => {
        const logsFile = process.env.LOGS_FILE_PATH;

        const allLines = readFileSync(logsFile, "utf-8").trim().split("***");

        peer.send(JSON.stringify({ lines: allLines }));

        let flag = allLines.length;

        watchLogFile(logsFile, (data) => {
            const lines = data.trim().split("***");
            
            peer.send(
                JSON.stringify({ lines: lines.slice(flag - lines.length) })
            );

            flag = lines.length;
        });
    },
});

function watchLogFile(path, onChange) {
    watchFile(path, { interval: 1000 }, () => {
        const content = readFileSync(path, "utf-8");
        onChange(content);
    });
}
