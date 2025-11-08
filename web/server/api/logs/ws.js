import { watchFile, readFileSync } from "fs";

const filePath = "/tmp/nvr/logs";

export default defineWebSocketHandler({
    open: (peer) => {
        const allLines = readFileSync(filePath, "utf-8").trim().split("\n");
        peer.send(JSON.stringify({ lines: allLines }));

        let flag = allLines.length;

        watchLogFile(filePath, (data) => {
            const lines = data.trim().split("\n");

            peer.send(
                JSON.stringify({ lines: lines.slice(flag - lines.length) })
            );

            flag = lines.length;
        });
    },
});

function watchLogFile(filePath, onChange) {
    watchFile(filePath, { interval: 1000 }, () => {
        const content = readFileSync(filePath, "utf-8");
        onChange(content);
    });
}
