//
// Entity|Component|System implementation in C++
//
// Copyright (c) 2015 Vittorio Romeo
// License: AFL 3.0 | https://opensource.org/licenses/AFL-3.0
// http://vittorioromeo.info | vittorio.romeo@outlook.com
//
// CppCon2015 talk about ECS and the implementation: https://www.youtube.com/watch?v=NTWSeQtHZ9M
// Slides of this talk, and original source code: http://bit.ly/2bfEoxF
//
// Re-packaged in the current form by https://github.com/netgusto
// Source-code for this library: https://github.com/netgusto/ecs
//

#ifndef ecs_ecs_hpp
#define ecs_ecs_hpp

#include <cassert>
#include <cstring>
#include <vector>
#include <bitset>

#include "./MPL/MPL.hpp"

// In this segment we'll implement the simple shoot'em'up game using
// our new component-based entity system.

namespace ecs {

    MPL_STRONG_TYPEDEF(std::size_t, DataIndex);
    MPL_STRONG_TYPEDEF(std::size_t, EntityIndex);
    MPL_STRONG_TYPEDEF(std::size_t, HandleDataIndex);
    MPL_STRONG_TYPEDEF(int, Counter);

    template <typename... Ts>
    using Signature = MPL::TypeList<Ts...>;

    template <typename... Ts>
    using SignatureList = MPL::TypeList<Ts...>;

    template <typename... Ts>
    using ComponentList = MPL::TypeList<Ts...>;

    template <typename... Ts>
    using TagList = MPL::TypeList<Ts...>;

    namespace Impl {

        template <typename TSettings>
        struct Entity {
            using Settings = TSettings;
            using Bitset = typename Settings::Bitset;

			DataIndex dataIndex;
            HandleDataIndex handleDataIndex;
            Bitset bitset;
            bool alive;
        };

        struct HandleData {
            EntityIndex entityIndex;
            Counter counter;
        };

        struct Handle {
            HandleDataIndex handleDataIndex;
            Counter counter;
        };

        template <typename TSettings>
        class ComponentStorage {

            public:
                void grow(std::size_t mNewCapacity) {
                    Utils::forTuple(
                        [this, mNewCapacity](auto& v) {
                            v.resize(mNewCapacity);
                        },
                        vectors
                    );
                }

                template <typename T>
                auto& getComponent(DataIndex mI) noexcept {
                    return std::get<std::vector<T>>(vectors)[mI];
                }

            private:
                using Settings = TSettings;
                using ComponentList = typename Settings::ComponentList;

                template <typename... Ts>
                using TupleOfVectors = std::tuple<std::vector<Ts>...>;

                MPL::Rename<TupleOfVectors, ComponentList> vectors;
        };

        template <typename TSettings>
        struct SignatureBitsets {

            using Settings = TSettings;
            using ThisType = SignatureBitsets<Settings>;
            using SignatureList = typename Settings::SignatureList;
            using Bitset = typename Settings::Bitset;

            using BitsetStorage = MPL::Tuple<MPL::Repeat<Settings::signatureCount(), Bitset>>;

            template <typename T>
            using IsComponentFilter = std::integral_constant<bool, Settings::template isComponent<T>()>;

            template <typename T>
            using IsTagFilter = std::integral_constant<bool, Settings::template isTag<T>()>;

            template <typename TSignature>
            using SignatureComponents = MPL::Filter<IsComponentFilter, TSignature>;

            template <typename TSignature>
            using SignatureTags = MPL::Filter<IsTagFilter, TSignature>;
        };

        template <typename TSettings>
        class SignatureBitsetsStorage {
            public:

                SignatureBitsetsStorage() noexcept {
                    MPL::forTypes<SignatureList>([this](auto t) {
                        this->initializeBitset<typename decltype(t)::type>();
                    });
                }

                template <typename T>
                auto& getSignatureBitset() noexcept {
                    static_assert(Settings::template isSignature<T>(), "");
                    return std::get<Settings::template signatureID<T>()>(storage);
                }

                template <typename T>
                const auto& getSignatureBitset() const noexcept {
                    static_assert(Settings::template isSignature<T>(), "");
                    return std::get<Settings::template signatureID<T>()>(storage);
                }

            private:

                using Settings = TSettings;
                using SignatureBitsets = typename Settings::SignatureBitsets;
                using SignatureList = typename SignatureBitsets::SignatureList;
                using BitsetStorage = typename SignatureBitsets::BitsetStorage;

                BitsetStorage storage;

                template <typename T>
                void initializeBitset() noexcept {
                    auto& b(this->getSignatureBitset<T>());

                    using SignatureComponents = typename SignatureBitsets::template SignatureComponents<T>;

                    using SignatureTags = typename SignatureBitsets::template SignatureTags<T>;

                    MPL::forTypes<SignatureComponents>([this, &b](auto t) {
                        b[Settings::template componentBit<typename decltype(t)::type>()] = true;
                    });

                    MPL::forTypes<SignatureTags>([this, &b](auto t) {
                        b[Settings::template tagBit<typename decltype(t)::type>()] = true;
                    });
                }

        };
    }

    template <typename TComponentList, typename TTagList, typename TSignatureList>
    struct Settings {

        using ComponentList = typename TComponentList::TypeList;
        using TagList = typename TTagList::TypeList;
        using SignatureList = typename TSignatureList::TypeList;
        using ThisType = Settings<ComponentList, TagList, SignatureList>;

        using SignatureBitsets = Impl::SignatureBitsets<ThisType>;
        using SignatureBitsetsStorage = Impl::SignatureBitsetsStorage<ThisType>;

        template <typename T>
        static constexpr bool isComponent() noexcept {
            return MPL::Contains<T, ComponentList>{};
        }

        template <typename T>
        static constexpr bool isTag() noexcept {
            return MPL::Contains<T, TagList>{};
        }

        template <typename T>
        static constexpr bool isSignature() noexcept {
            return MPL::Contains<T, SignatureList>{};
        }

        static constexpr std::size_t componentCount() noexcept {
            return MPL::size<ComponentList>();
        }

		static constexpr std::size_t component_count = MPL::size<ComponentList>();
		static constexpr std::size_t tag_count = MPL::size<TagList>();

        static constexpr std::size_t tagCount() noexcept {
            return MPL::size<TagList>();
        }

        static constexpr std::size_t signatureCount() noexcept {
            return MPL::size<SignatureList>();
        }

        template <typename T>
        static constexpr std::size_t componentID() noexcept {
            return MPL::IndexOf<T, ComponentList>{};
        }

        template <typename T>
        static constexpr std::size_t tagID() noexcept {
            return MPL::IndexOf<T, TagList>{};
        }

        template <typename T>
        static constexpr std::size_t signatureID() noexcept {
            return MPL::IndexOf<T, SignatureList>{};
        }

        using Bitset = std::bitset<component_count + tag_count>;

        template <typename T>
        static constexpr std::size_t componentBit() noexcept {
            return componentID<T>();
        }

        template <typename T>
        static constexpr std::size_t tagBit() noexcept {
            return componentCount() + tagID<T>();
        }
    };

    template <typename TSettings>
    class Manager {

        public:
            using Handle = Impl::Handle;

            Manager() { growTo(100); }

            auto isHandleValid(const Handle& mX) const noexcept {
                return mX.counter == getHandleData(mX).counter;
            }

            auto getEntityIndex(const Handle& mX) const noexcept {
                assert(isHandleValid(mX));
                return getHandleData(mX).entityIndex;
            }

            auto isAlive(EntityIndex mI) const noexcept {
                return getEntity(mI).alive;
            }

            auto isAlive(const Handle& mX) const noexcept {
                return isAlive(getEntityIndex(mX));
            }

            void kill(EntityIndex mI) noexcept { getEntity(mI).alive = false; }
            void kill(const Handle& mX) noexcept { kill(getEntityIndex(mX)); }

            template <typename T>
            auto hasTag(EntityIndex mI) const noexcept {
                static_assert(Settings::template isTag<T>(), "");
                return getEntity(mI).bitset[Settings::template tagBit<T>()];
            }

            template <typename T>
            auto hasTag(const Handle& mX) const noexcept {
                return hasTag<T>(getEntityIndex(mX));
            }

            template <typename T>
            void addTag(EntityIndex mI) noexcept {
                static_assert(Settings::template isTag<T>(), "");
                getEntity(mI).bitset[Settings::template tagBit<T>()] = true;
            }

            template <typename T>
            void addTag(const Handle& mX) noexcept {
                return addTag<T>(getEntityIndex(mX));
            }

            template <typename T>
            void delTag(EntityIndex mI) noexcept {
                static_assert(Settings::template isTag<T>(), "");
                getEntity(mI).bitset[Settings::template tagBit<T>()] = false;
            }

            template <typename T>
            auto delTag(const Handle& mX) noexcept {
                return delTag<T>(getEntityIndex(mX));
            }

            template <typename T>
            auto hasComponent(EntityIndex mI) const noexcept {
                static_assert(Settings::template isComponent<T>(), "");
                return getEntity(mI).bitset[Settings::template componentBit<T>()];
            }

            template <typename T>
            auto hasComponent(const Handle& mX) const noexcept {
                return hasComponent<T>(getEntityIndex(mX));
            }

            template <typename T, typename... TArgs>
            auto& addComponent(EntityIndex mI, TArgs&&... mXs) noexcept {
                static_assert(Settings::template isComponent<T>(), "Component not recognized: ");

                auto& e(getEntity(mI));
                e.bitset[Settings::template componentBit<T>()] = true;

                auto& c(components.template getComponent<T>(e.dataIndex));
                new(&c) T(MPL_FWD(mXs)...);
                //new(&c) T(::std::forward<decltype(mXs)...>(mXs)...);

                return c;
            }

            template <typename T, typename... TArgs>
            auto& addComponent(const Handle& mX, TArgs&&... mXs) noexcept {
                return addComponent<T>(getEntityIndex(mX), MPL_FWD(mXs)...);
            }

            template <typename T>
            auto& getComponent(EntityIndex mI) noexcept {
                static_assert(Settings::template isComponent<T>(), "");
                assert(hasComponent<T>(mI));

                return components.template getComponent<T>(getEntity(mI).dataIndex);
            }

            template <typename T>
            auto& getComponent(const Handle& mX) noexcept {
                return getComponent<T>(getEntityIndex(mX));
            }

            template <typename T>
            void delComponent(EntityIndex mI) noexcept {
                static_assert(Settings::template isComponent<T>(), "");
                getEntity(mI).bitset[Settings::template componentBit<T>()] = false;
            }

            template <typename T>
            void delComponent(const Handle& mX) noexcept {
                return delComponent<T>(getEntityIndex(mX));
            }

            auto createIndex() {
                growIfNeeded();

                EntityIndex freeIndex(sizeNext++);

                assert(!isAlive(freeIndex));
                auto& e(entities[freeIndex]);
                e.alive = true;
                e.bitset.reset();

                return freeIndex;
            }

            auto createHandle() {

                auto freeIndex(createIndex());
                assert(isAlive(freeIndex));

                auto& e(entities[freeIndex]);
                auto& hd(handleData[e.handleDataIndex]);
                hd.entityIndex = freeIndex;

                Handle h;
                h.handleDataIndex = e.handleDataIndex;
                h.counter = hd.counter;
                assert(isHandleValid(h));

                return h;
            }

            void clear() noexcept {

                for(auto i(0u); i < capacity; ++i) {
                    auto& e(entities[i]);
                    auto& hd(handleData[i]);

                    e.dataIndex = i;
                    e.bitset.reset();
                    e.alive = false;
                    e.handleDataIndex = i;

                    hd.counter = 0;
                    hd.entityIndex = i;
                }

                size = sizeNext = 0;
            }

            void refresh() noexcept {

                if(sizeNext == 0) {
                    size = 0;
                    return;
                }

                size = sizeNext = refreshImpl();
            }

            template <typename T>
            auto inline matchesSignature(EntityIndex mI) const noexcept {
                static_assert(Settings::template isSignature<T>(), "");

                const auto& entityBitset(getEntity(mI).bitset);
                const auto& signatureBitset(signatureBitsets.template getSignatureBitset<T>());

                return (signatureBitset & entityBitset) == signatureBitset;
            }

            template <typename TF>
            void inline forEntities(TF&& mFunction) {
                for(EntityIndex i{0}; i < size; ++i) mFunction(i);
            }

            template <typename T, typename TF>
            void forEntitiesMatching(TF&& mFunction) {
                static_assert(Settings::template isSignature<T>(), "");

                forEntities([this, &mFunction](auto i) {
                    if(matchesSignature<T>(i)) expandSignatureCall<T>(i, mFunction);
                });
            }
        
            template <typename T>
            std::vector<EntityIndex> entitiesMatching() {
                static_assert(Settings::template isSignature<T>(), "");
                
                std::vector<EntityIndex> res{};

                for(EntityIndex i{0}; i < size; ++i) {
                    if(matchesSignature<T>(i)) {
                        res.emplace_back(i);
                    }
                }
                
                return std::forward<std::vector<EntityIndex>>(res);
            }

            auto getEntityCount() const noexcept { return size; }

            auto getCapacity() const noexcept { return capacity; }

            /*auto printState(std::ostream& mOSS) const {
                mOSS << "\nsize: " << size << "\nsizeNext: " << sizeNext << "\ncapacity: " << capacity << "\n";

                for(auto i(0u); i < sizeNext; ++i) {
                    auto& e(entities[i]);
                    mOSS << (e.alive ? "A" : "D");
                }

                mOSS << "\n\n";
            }*/

				auto& getHandleData(EntityIndex mI) noexcept {
					return getHandleData(getEntity(mI).handleDataIndex);
				}

				const auto& getHandleData(EntityIndex mI) const noexcept {
					return getHandleData(getEntity(mI).handleDataIndex);
				}

        private:
            using Settings = TSettings;
            using ThisType = Manager<Settings>;
            using Bitset = typename Settings::Bitset;
            using Entity = Impl::Entity<Settings>;
            using HandleData = Impl::HandleData;

            using SignatureBitsetsStorage = Impl::SignatureBitsetsStorage<Settings>;
            using ComponentStorage = Impl::ComponentStorage<Settings>;

            std::size_t capacity{0}, size{0}, sizeNext{0};
            std::vector<Entity> entities;
            SignatureBitsetsStorage signatureBitsets;
            ComponentStorage components;
            std::vector<HandleData> handleData;

            void growTo(std::size_t mNewCapacity) {

                assert(mNewCapacity > capacity);

                entities.resize(mNewCapacity);
                components.grow(mNewCapacity);
                handleData.resize(mNewCapacity);

                for(auto i(capacity); i < mNewCapacity; ++i) {
                    auto& e(entities[i]);
                    auto& h(handleData[i]);

                    e.dataIndex = i;
                    e.bitset.reset();
                    e.alive = false;
                    e.handleDataIndex = i;

                    h.counter = 0;
                    h.entityIndex = i;
                }

                capacity = mNewCapacity;
            }

            void growIfNeeded() {
                if(capacity > sizeNext) return;
                growTo((capacity + 10) * 2);
            }

            auto& getEntity(EntityIndex mI) noexcept {
                assert(sizeNext > mI);
                return entities[mI];
            }

            const auto& getEntity(EntityIndex mI) const noexcept {
                assert(sizeNext > mI);
                return entities[mI];
            }

            auto& getHandleData(HandleDataIndex mI) noexcept {
                assert(handleData.size() > mI);
                return handleData[mI];
            }

            const auto& getHandleData(HandleDataIndex mI) const noexcept {
                assert(handleData.size() > mI);
                return handleData[mI];
            }

            auto& getHandleData(const Handle& mX) noexcept {
                return getHandleData(mX.handleDataIndex);
            }

            const auto& getHandleData(const Handle& mX) const noexcept {
                return getHandleData(mX.handleDataIndex);
            }

            template <typename... Ts>
            struct ExpandCallHelper {
                template <typename TF>
                static void call(EntityIndex mI, ThisType& mMgr, TF&& mFunction) {
                    auto di(mMgr.getEntity(mI).dataIndex);
                    mFunction(mI, mMgr.components.template getComponent<Ts>(di)...);
                }
            };

            template <typename T, typename TF>
            void expandSignatureCall(EntityIndex mI, TF&& mFunction) {

                static_assert(Settings::template isSignature<T>(), "");

                using RequiredComponents = typename Settings::SignatureBitsets::template SignatureComponents<T>;
                using Helper = MPL::Rename<ExpandCallHelper, RequiredComponents>;

                Helper::call(mI, *this, mFunction);
            }



            void invalidateHandle(EntityIndex mX) noexcept {
                auto& hd(handleData[entities[mX].handleDataIndex]);
                ++hd.counter;
            }

            void refreshHandle(EntityIndex mX) noexcept {
                auto& hd(handleData[entities[mX].handleDataIndex]);
                hd.entityIndex = mX;
            }

            auto refreshImpl() noexcept {

                EntityIndex iD{0}, iA{sizeNext - 1};

                while(true) {
                    for(; true; ++iD) {
                        if(iD > iA) return iD;
                        if(!entities[iD].alive) break;
                    }

                    for(; true; --iA) {
                        if(entities[iA].alive) break;
                        invalidateHandle(iA);
                        if(iA <= iD) return iD;
                    }

                    assert(entities[iA].alive);
                    assert(!entities[iD].alive);

                    std::swap(entities[iA], entities[iD]);

                    refreshHandle(iD);

                    invalidateHandle(iA);
                    refreshHandle(iA);

                    ++iD;
                    --iA;
                }

                return iD;
            }
    };
}

#endif /* ecs_ecs_hpp */
