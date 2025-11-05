<script setup>
const { t } = useI18n()
const route = useRoute()
const router = useRouter()

const items = computed(() => [
    {
        name: 'Dashboard',
        label: t('dashboard'),
        icon: 'pi pi-table',
        command: function () { router.push('/') }
    },
    {
        name: 'Files',
        label: t('files'),
        icon: 'pi pi-folder',
        command: function () { router.push('/files') }
    },
    {
        name: 'Cameras',
        label: t('cameras'),
        icon: 'pi pi-camera',
        command: function () { router.push('/cameras') }
    },
    {
        name: 'Settings',
        label: t('settings'),
        icon: 'pi pi-cog',
        command: function () { router.push('/settings') }
    },
].map(item => {
    route.name == item.name && (item.class = 'bg-surface-50');
    return item;
}));
</script>

<template>
    <aside class="self-stretch w-80 rounded bg-white p-4 shadow-sm">
        <PanelMenu :model="items" />
    </aside>

</template>
