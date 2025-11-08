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
        name: 'logs',
        label: t('logs'),
        icon: 'pi pi-list',
        command: function () { router.push('/logs') }
    },
].map(item => {
    route.name == item.name && (item.class = 'bg-surface-50');
    return item;
}));

const { pending, data, refresh } = useFetch('/api/files/memory')

</script>

<template>
    <aside class="flex self-stretch w-80 rounded bg-white p-4 shadow-sm">
        <div class=" flex flex-col justify-between gap-6 grow">
            <PanelMenu :model="items" />
            <div class="flex flex-col items-center h-max border-t border-gray-100 pt-4 relative">
                <Knob :model-value="data?.used || 0" :max="data?.size" ReadOnly :size="200" :strokeWidth="6" class="ltr"
                    :valueTemplate="`{value} / ${data?.size || 0} GB`"
                    pt:text="text-[7px] -translate-y-2 font-['arial']" />
                <Button icon="pi pi-refresh" text rounded severity="secondary" class="absolute! top-36"
                    :loading="pending" @click="refresh()" />
                <span class="text-lg">{{ $t('sd-card') }}</span>
            </div>
        </div>
    </aside>
</template>
