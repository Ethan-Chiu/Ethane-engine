#include "Spin.h"

#include "Ethane/Schema/TypeRegistry.h"
#include "Ethane/World/Components.h"
#include "Ethane/World/Script.h"

namespace Ethane {

	namespace {

		class SpinScript : public Script
		{
		public:
			void OnUpdate(float deltaTime) override
			{
				TransformComponent* transform = Get<TransformComponent>();
				if (!transform) {
					return;
				}
				const SpinComponent* spin = Get<SpinComponent>();
				transform->Rotation.y += (spin ? spin->Speed : 0.5f) * deltaTime;
			}
		};

	}

	void RegisterGameTypes()
	{
		TypeRegistry& types = TypeRegistry::Global();
		if (types.Find<SpinComponent>()) {
			return;
		}
		types.Register<SpinComponent>("Spin")
			.Field("Speed", &SpinComponent::Speed, FieldFlags::None, { .Speed = 0.01f });
		ScriptRegistry::Global().Register<SpinScript>("Spin");
	}

}
