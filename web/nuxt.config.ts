import tailwindcss from "@tailwindcss/vite";

// https://nuxt.com/docs/api/configuration/nuxt-config
export default defineNuxtConfig({
    compatibilityDate: '2025-07-15',
    devtools: { enabled: false },
    ssr: false,
    sourcemap: false,
    css: ['./app/assets/css/main.css'],
    modules: ['@primevue/nuxt-module', '@nuxtjs/i18n', 'nuxt-auth-utils'],
    primevue: {
        options: {
            ripple: true,

        },
        importPT: { from: '@/themes/pt.ts', as: 'passthrough' },
        importTheme: { from: '@/themes/index.ts', as: 'myTheme' },
        components: {
            include: [
                'Button',
                'DataTable',
                'InputSwitch',
                'InputText',
                'Password',
                'Menu',
                'DynamicDialog',
                'PanelMenu',
                'Knob',
                'FloatLabel',
                'Toast'
            ]
        }
    },
    i18n: {
        locales: [
            { code: 'fa', language: 'fa-IR', file: 'fa.json' },
        ],
        defaultLocale: 'fa',
        langDir: '../app/locales',
        defaultDirection: 'rtl',
        detectBrowserLanguage: false,
        strategy: 'no_prefix'
    },
    vite: {
        plugins: [
            tailwindcss(),
        ],
    },
    nitro: {
        experimental: {
            websocket: true
        }
    },
    build: {
        analyze: true,
    },
})