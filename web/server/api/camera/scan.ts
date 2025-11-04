import { execFile } from 'child_process';
import { promisify } from 'util';
import { join } from 'path'

const execFileAsync = promisify(execFile);

let timer: any;
let runnig = false;
let script = join(process.cwd(), '../scripts/scan')

const command = async () => {
    try {
        const { stdout, stderr } = await execFileAsync(script)
        if (stderr) console.error(stderr)

        console.log(stdout);

    } catch (err) {
        console.error(err)
        runnig = false
    }

    if (runnig) timer = setTimeout(command, 1000);
}

export default defineEventHandler((event) => {
    const { enabled = false } = getQuery(event)
    
    if (enabled) {
        if (runnig) return { enabled }
        runnig = true
        command()
    } else if (runnig) {
        runnig = false
        clearTimeout(timer)
    }

    return { enabled }
})