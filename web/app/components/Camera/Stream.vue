<script setup>
const dialog = inject('dialogRef')

const { url } = dialog.value.data

const img = ref()

onBeforeUnmount(() => img.value.src = '')
</script>

<template>
    <div class="flex flex-col gap-4">
        <img ref="img" controls autoplay muted playsinline width="1080" height="640" :src="url">
        <div class="flex items-center justify-between">
            <Button :label="$t('back')" severity="secondary" outlined @click="dialog.close()" />
        </div>
    </div>
</template>