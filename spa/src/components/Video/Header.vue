<script setup>
const store = useFileStore()

const emit = defineEmits(['refresh'])

const direction = defineModel()

const history = ref([])

watch(() => direction.value.length, (v, o) => {
    v > o && history.value.pop()
})
</script>

<template>
    <div class="flex items-center gap-2">
        <span class="text-2xl font-bold pl-4">{{ $t('videos') }}</span>
        <InputText class="grow ltr" readonly :value="['cameras', ...direction].join('/')" />
        <Button icon="pi pi-refresh" rounded text severity="secondary" :loading="store.fetching"
            @click="store.index(direction.join())" />
        <Button icon="pi pi-chevron-right" text rounded severity="secondary" :disabled="history.length < 1 || fetching"
            @click="direction.push(history.pop())" />
        <Button icon="pi pi-chevron-left" text rounded severity="secondary" :disabled="direction.length == 0 || fetching"
            @click="history.push(direction.pop())" />
    </div>
</template>
