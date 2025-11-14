export const useFileStore = defineStore('file', {
    state: () => ({
        fetching: false,
        scanning: false,
        items: [],
        memory: { size: 0, used: 0 }
    }),
    actions: {
        async index(direction) {
            this.fetching = true

            const { status, data } = await this.axios.get('/files', { params: { direction } })

            this.fetching = false

            if (status === 200)
                this.items = data
        },
        async getMemomry() {
            this.scanning = true

            const { status, data } = await this.axios.post('/files/memory')

            this.scanning = false

            if (status === 200) {
                this.memory = data
            }
        }
    }
})