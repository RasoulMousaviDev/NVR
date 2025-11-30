export const useCameraStore = defineStore('camera', {
    state: () => ({
        fetching: false,
        scanning: false,
        loading: false,
        items: []
    }),
    actions: {
        async index() {
            this.fetching = true

            const { status, data } = await this.axios.get('/cameras')

            this.fetching = false

            if (status === 200)
                this.items = data.items

        },
        async scan() {
            this.scanning = true

            const { status } = await this.axios.post('/cameras/scan')

            this.scanning = false

            if (status === 200)
                this.index()
        },
        async settings(id, form) {
            this.loading = true

            const data = new URLSearchParams(form)
            const { status } = await this.axios.post(`/cameras/${id}/settings`, data)

            this.loading = false

            return status;
        },
        async credentials(id, form) {
            this.loading = true

            const data = new URLSearchParams(form)
            const { status } = await this.axios.post(`/cameras/${id}/credentials`, data)

            this.loading = false

            return status;
        },
        async record(id, value) {
            this.loading = true

            const res = await this.axios[value ? 'post' : 'delete'](`/cameras/${id}/record`, {})

            this.loading = false

            return res;
        },
        async stream(id, value) {
            return await this.axios[value ? 'post' : 'delete'](`/cameras/${id}/stream`, {})
        },
    }

})
