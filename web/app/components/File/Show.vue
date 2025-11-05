<script setup>
const dialog = inject('dialogRef')

const { file, direction } = dialog.value.data

const url = `/api/files/show/${direction.join('/')}/${file.name}`
</script>

<template>
    <div class="flex flex-col gap-4">
        <video :src="url" autoplay controls width="1080" class="rounded-lg"></video>
        <div class="flex items-center justify-between">
            <Button :label="$t('back')" severity="secondary" outlined @click="dialog.close()" />
        </div>
    </div>
</template>