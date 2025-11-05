<script setup>
const dialog = inject('dialogRef')

const data = dialog.value.data

const url = `/api/cameras/${data.id}/stream`
</script>

<template>
    <div class="flex flex-col gap-4">
        <img controls autoplay muted playsinline width="1080" height="640" :src="url"></img>
        <div class="flex items-center justify-between">
            <Button :label="$t('back')" severity="secondary" outlined @click="dialog.close()" />
        </div>
    </div>
</template>