<script setup>
useHead({ htmlAttrs: { dir: 'rtl' } })

useSeoMeta({ title: 'NVR Control' })
</script>

<template>
    <NuxtLayout>
        <NuxtPage />
        <DynamicDialog />
        <Toast position="bottom-left" />
    </NuxtLayout>
</template>
