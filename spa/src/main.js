import './assets/css/main.css'

import { createApp } from 'vue'
import { createPinia } from 'pinia'
import { createI18n } from 'vue-i18n'


import App from './App.vue'
import router from './router'
import PrimeVue from 'primevue/config'
import fa from './locales/fa.json'
import ConfirmationService from "primevue/confirmationservice";
import DialogService from "primevue/dialogservice";
import ToastService from "primevue/toastservice";
import theme from './theme';
import pt from './theme/passthrogh'
import Axios from "axios";

const baseURL = "/cgi-bin/api";

const axios = Axios.create({ baseURL });

const i18n = createI18n({ legacy: false, locale: "fa", messages: { fa } });

const pinia = createPinia();
pinia.use(({ store }) => {
    store.axios = axios;
});

const app = createApp(App)
app.use(router)
app.use(pinia)
app.use(i18n)
app.use(ToastService);
app.use(DialogService);
app.use(ConfirmationService);
app.use(PrimeVue, { theme, pt, ripple: true })

app.mount('#app')


axios.interceptors.request.use((config) => config);

axios.interceptors.response.use(
    (res) => {
        return res;
    },
    (err) => {
        if (err?.response?.status === 401 && location != '/login') {
            router.push("/login");
        }

        return err.response || {};
    }
);