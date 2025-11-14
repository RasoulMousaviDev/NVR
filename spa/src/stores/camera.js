export const useCameraStore = defineStore('camera', {
    state: () => ({
        fetching: false,
        scanning: false,
        items: []
    }),
    actions: {
        index() {

        },
        scan() {
            this.scanning = true

            const { status } = this.axios.post('/cameras/scan')

            this.scanning = false

            return status;
        }
    }

})
