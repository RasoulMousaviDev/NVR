<script setup>
useHead({
    htmlAttrs: {
        dir: 'rtl'
    }
})
</script>

<template>
    <NuxtLayout>
        <NuxtPage />
        <DynamicDialog />
        <Toast position="bottom-left"/>
    </NuxtLayout>
</template>
