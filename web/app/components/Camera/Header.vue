<script setup>
defineProps(['fetching'])
const emit = defineEmits(['refresh'])

const { status, pending, execute } = useFetch('/api/cameras/scan', { method: 'post', immediate: false })

watch(pending, (v) => !v && status.value == 'success' && emit('refresh', ''))
</script>

<template>
    <div class="flex items-center gap-2">
        <span class="text-2xl font-bold">{{ $t('cameras') }}</span>
        <Button icon="pi pi-refresh" rounded text :loading="fetching" @click="emit('refresh', '')" />
        <hr>
        <Button :label="$t('scan')" icon="pi pi-search" severity="warn" :loading="pending"
            :disabled="fetching | pending" @click="execute()" />
        <Button :label="$t('new-camera')" icon="pi pi-plus" />
    </div>
</template>