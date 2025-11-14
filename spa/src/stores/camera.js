export const useCameraStore = defineStore('camera', {
    state: () => ({
        fetching: false,
        scanning: false,
        items: []
    }),
    actions: {
        async index() {
            this.fetching = true

            const { status, data } = await this.axios.post('/cameras')

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
        }
    }

})
