export const useAuthStore = defineStore('auth', {
    state: () => ({
        loading: false,
        token: ''
    }),
    actions: {
        async login(form) {
            const body = new URLSearchParams(form)

            this.loading = true

            const { status, data } = await this.axios.post('/login', body)

            this.loading = false

            if (status === 200)
                this.token = data.token

            return { status, data }
        },
    }
})
