<script setup>
const { t } = useI18n()
const route = useRoute()
const router = useRouter()

const items = computed(() => [
    {
        name: 'videos',
        label: t('videos'),
        icon: 'pi pi-video',
        command: function () { router.push('/videos') }
    },
    {
        name: 'cameras',
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

const value = ref(0)

const { pending, data, refresh } = useFetch('/api/files/memory')

</script>

<template>
    <aside class="flex self-stretch w-80 rounded bg-white p-4 shadow-sm">
        <div class=" flex flex-col justify-between gap-6 grow">
            <PanelMenu :model="items" />
            <div class="flex flex-col items-center h-max border-t border-gray-100 pt-4 relative">
                <Knob :model-value="data?.value || 0" ReadOnly :size="180" :strokeWidth="7" class="ltr"
                    :valueTemplate="`{value} / ${data?.total || 0} GB`"
                    pt:text="text-[9px] -translate-y-2 font-['arial']" />
                <Button icon="pi pi-refresh" text rounded severity="secondary" class="absolute! top-36" :loading="pending" @click="refresh()" />
                <span class="text-lg">{{ $t('sd-card') }}</span>
            </div>
        </div>
    </aside>
</template>
