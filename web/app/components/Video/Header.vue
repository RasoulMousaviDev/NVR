<script setup>
defineProps(['fetching'])

const emit = defineEmits(['refresh'])

const folders = defineModel()

const history = ref([])

watch(() => folders.value.length, (v, o) => {
    v > o && history.value.pop()
})
</script>

<template>
    <div class="flex items-center gap-2">
        <span class="text-2xl font-bold pl-4">{{ $t('videos') }}</span>
        <InputText class="grow ltr" readonly :value="folders.join('/')" />
        <Button icon="pi pi-refresh" rounded text severity="secondary" :loading="fetching"
            @click="emit('refresh', true)" />
        <Button icon="pi pi-chevron-right" text rounded severity="secondary" :disabled="history.length < 1 || fetching"
            @click="folders.push(history.pop())" />
        <Button icon="pi pi-chevron-left" text rounded severity="secondary" :disabled="folders.length <= 1 || fetching"
            @click="history.push(folders.pop())" />
    </div>
</template>
