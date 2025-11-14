import { createRouter, createWebHistory } from 'vue-router'

const router = createRouter({
    history: createWebHistory(import.meta.env.BASE_URL),
    routes: [
        {
            name: 'Login',
            path: '/login',
            component: () => import('@/views/Login.vue')
        },
        {
            name: "Panel",
            path: "/",
            redirect: "/cameras",
            component: () => import("@/layouts/Default.vue"),
            children: [
                {
                    name: "Videos",
                    path: "/videos",
                    component: import('@/views/Videos.vue')
                },
                {
                    name: "Cameras",
                    path: "/cameras",
                    component: import('@/views/Cameras.vue')
                }
            ]
        }
    ],
})

export default router
