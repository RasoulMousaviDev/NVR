export const useLogStore = defineStore('log', {
    state: () => ({

        items: [],
    }),
    actions: {
        async index(line) {
            const params = { line: line || undefined }

            const { status, data } = await this.axios.get('/logs', { params })

            if (status === 200) {
                if (typeof data === 'object')
                    this.items.push(data)
                else
                    this.items.push(...data.split("\n").filter(Boolean).map(JSON.parse))
            }
        },
    }
})