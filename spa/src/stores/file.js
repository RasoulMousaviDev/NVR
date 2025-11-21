export const useFileStore = defineStore('file', {
    state: () => ({
        fetching: false,
        scanning: false,
        items: [],
        storage: { size: 0, used: 0 }
    }),
    actions: {
        async index(direction) {
            const params = { direction: direction || undefined }
            
            this.fetching = true

            const { status, data } = await this.axios.get('/files', { params })

            this.fetching = false

            if (status === 200)
                this.items = data
        },
        async getStorage() {
            this.scanning = true

            const { status, data } = await this.axios.post('/files/storage')

            this.scanning = false

            if (status === 200) {
                this.storage = data
            }
        }
    }
})