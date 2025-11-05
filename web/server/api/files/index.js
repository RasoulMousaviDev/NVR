import { readdirSync, statSync } from 'node:fs'
import { join } from 'node:path'

export default defineEventHandler(async (event) => {
    let { direction = ['cameras'] } = getQuery(event);
    if(typeof direction !== 'object')
        direction = [direction]
    
    const baseDir = "/tmp/";
    const safeDir = baseDir + direction.filter(d => !d.includes('..')).join('/')
   
    
    const items = readdirSync(safeDir).map((name) => {
        const fullPath = join(safeDir, name);
        const stats = statSync(fullPath);
        const type = stats.isDirectory() ? "folder" : "file";

        return { name, type };
    });

    return { items };
});
