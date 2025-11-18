<script setup>
const dialog = inject('dialogRef')

const { file, direction } = dialog.value.data

const url = `http://192.168.1.100:3000/cgi-bin/api/files/${direction.join('-')}-${file.name.slice(0, 5)}/play`
</script>

<template>
    <div class="flex flex-col gap-4">
        <video :src="url" autoplay controls width="1080" class="rounded-lg"></video>
        <div class="flex items-center justify-between">
            <Button :label="$t('back')" severity="secondary" outlined @click="dialog.close()" />
        </div>
    </div>
</template>