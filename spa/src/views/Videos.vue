<script setup>
definePageMeta({ middleware: ['auth'] })

const dialog = useDialog()

const folders = ref([])

const route = useRoute()

if(route.query.camera)
    folders.value.push(route.query.camera)

const { data, pending, refresh } = await useFetch('/api/files', {
    query: { direction: folders.value },
    watch: [() => folders.value.length]
})

const VideoPlay = defineAsyncComponent(() => import('~/components/Video/Play.vue'))

const handleClick = (item) => {
    if (item.type == 'folder') {
        folders.value.push(item.name)
    } else {
        dialog.open(VideoPlay, {
            props: { modal: true, closable: true, header: item.name },
            data: { direction: folders.value, file: item }
        })
    }
}


</script>

<template>
    <div class="bg-white rounded p-6 h-full flex flex-col gap-4">
        <VideoHeader v-model="folders" :fetching="pending" @refresh="refresh" />
        <div :class="{ 'pointer-events-none': pending }" class="shadow-inner h-full rounded p-6 ltr">
            <ul class="flex flex-wrap gap-x-5 gap-y-8 ">
                <li v-for="(item, key) in data?.items" :key class="h-max">
                    <Button :label="item.name" :icon="`pi pi-${item.type}`" icon-pos="top" text severity="secondary"
                        :pt="{ root: 'w-32 h-32', icon: 'text-7xl', label: 'text-sm w-28' }"
                        @click="handleClick(item)" />
                </li>
                <li v-if="data?.items.length === 0 && !pending" class="mx-auto">
                    <p class="text-center text-sm opacity-60">
                        {{ $t('not-found') }}
                    </p>
                </li>
            </ul>
        </div>
    </div>
</template>