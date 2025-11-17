import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import vueDevTools from 'vite-plugin-vue-devtools'
import Components from 'unplugin-vue-components/vite'
import AutoImport from 'unplugin-auto-import/vite'
import { PrimeVueResolver } from '@primevue/auto-import-resolver'
import tailwindcss from '@tailwindcss/vite'

// https://vite.dev/config/
export default defineConfig({
    plugins: [
        vue(),
        vueDevTools(),
        tailwindcss(),
        Components({
            resolvers: [PrimeVueResolver()],
            extensions: ['vue'],
            directoryAsNamespace: true,
            deep: true,
            dts: 'src/types/components.d.ts'
        }),
        AutoImport({
            imports: [
                'vue',
                'vue-router',
                'pinia',
            ],
            dirs: [
                'src/composables',
                'src/stores',
            ],
            vueTemplate: true,
            dts: 'src/types/auto-imports.d.ts',
        }),
    ],
    resolve: {
        alias: {
            '@': fileURLToPath(new URL('./src', import.meta.url)),
        },
    },
    server: {
        proxy: {
            "/cgi-bin/api": {
                target: 'http://192.168.1.100:3000',
                changeOrigin: true,
                secure: false,
                rewrite: (path) => path.replace(/^\/cgi-bin\/api/, "/cgi-bin/api"),
            }
        },
        cors: true
    }
})
