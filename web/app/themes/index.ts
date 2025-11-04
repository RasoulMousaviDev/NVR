
import { definePreset } from '@primeuix/themes';
import Aura from '@primeuix/themes/aura';

const preset = definePreset(Aura, {
    semantic: {
        primary: {
            50: '{surface.50}',
            100: '{surface.100}',
            200: '{surface.200}',
            300: '{surface.300}',
            400: '{surface.400}',
            500: '{surface.500}',
            600: '{surface.600}',
            700: '{surface.700}',
            800: '{surface.800}',
            900: '{surface.900}',
            950: '{surface.950}'
        },
        colorScheme: {
            light: {
                primary: {
                    color: '{primary.950}',
                    contrastColor: '#ffffff',
                    hoverColor: '{primary.800}',
                    activeColor: '{primary.700}',
                },
                highlight: {
                    background: '{primary.50}',
                    focusBackground: '{primary.100}',
                    color: '{primary.700}',
                    focusColor: '{primary.800}'
                },
            },
            dark: {
                primary: {
                    color: '{primary.50}',
                    contrastColor: '{primary.950}',
                    hoverColor: '{primary.200}',
                    activeColor: '{primary.300}'
                },
                highlight: {
                    background: 'color-mix(in srgb, {primary.400}, transparent 84%)',
                    focusBackground: 'color-mix(in srgb, {primary.400}, transparent 76%)',
                    color: 'rgba(255,255,255,.87)',
                    focusColor: 'rgba(255,255,255,.87)'
                }
            }
        }
    }
});

export default {
    preset,
    options: {
        darkModeSelector: '.dark',
        cssLayer: {
            name: 'primevue',
            order: 'theme, base, primevue'
        }
    }
};