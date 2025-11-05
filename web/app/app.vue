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
    </NuxtLayout>
    <DynamicDialog />
</template>
