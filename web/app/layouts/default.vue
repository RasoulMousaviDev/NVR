<script setup>

</script>

<template>
    <div class="flex flex-col bg-gray-50 min-h-screen">
        <AppToolbar/>
        <div class="flex gap-8 p-8 grow">
            <AppSidebar />
            <main class="grow">
                <slot></slot>
            </main>
        </div>
    </div>
</template>