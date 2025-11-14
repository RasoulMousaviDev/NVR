<script setup>
const store = useCameraStore()

</script>

<template>
    <div class="flex items-center gap-2">
        <span class="text-2xl font-bold">{{ $t('cameras') }}</span>
        <Button icon="pi pi-refresh" rounded text severity="secondary" :loading="store.fetching"
            @click="store.index()" />
        <hr>
        <Button :label="$t('scan')" icon="pi pi-search" severity="warn" :loading="store.scanning"
            :disabled="store.fetching || store.scanning" @click="store.scan()" />
        <!-- <Button :label="$t('new-camera')" icon="pi pi-plus" /> -->
    </div>
</template>