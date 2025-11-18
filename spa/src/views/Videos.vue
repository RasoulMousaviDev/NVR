<script setup>
const { dialog, route } = inject('services')

const store = useFileStore()

const direction = ref([])

if (route.query.camera)
    direction.value.push(route.query.camera)

watch(direction, (v) => store.index(v.join()), { immediate: true, deep: true })

const VideoPlay = defineAsyncComponent(() => import('@/components/Video/Play.vue'))

const handleClick = (item) => {
    if (item.type == 'folder') {
        direction.value.push(item.name)
    } else {
        dialog.open(VideoPlay, {
            props: { modal: true, closable: true, header: item.name },
            data: { direction: direction.value, file: item }
        })
    }
}


</script>

<template>
    <div class="bg-white rounded p-6 h-full flex flex-col gap-4">
        <VideoHeader v-model="direction" />
        <div :class="{ 'pointer-events-none': pending }" class="shadow-inner h-full rounded p-6 ltr">
            <ul class="flex flex-wrap gap-x-5 gap-y-8 ">
                <li v-for="(item, key) in store.items" :key class="h-max">
                    <Button :label="item.name" :icon="`pi pi-${item.type}`" icon-pos="top" text severity="secondary"
                        :pt="{ root: 'w-32 h-32', icon: 'text-7xl', label: 'text-sm w-28' }"
                        @click="handleClick(item)" />
                </li>
                <li v-if="store.items.length === 0 && !pending" class="mx-auto">
                    <p class="text-center text-sm opacity-60">
                        {{ $t('not-found') }}
                    </p>
                </li>
            </ul>
        </div>
    </div>
</template>